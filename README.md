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
| 1| Constant    | OpAb  | b <- constants\[A\]                                 |
| 2| Not         | OpAB  | B <- !A                                             |
| 3| NegateI     | OpAB  | B <- -A                                             |
| 4| NegateF     | OpAB  | B <- -A                                             |
| 5| AddI        | Opabc | c <- a + b                                          |
| 6| SubI        | Opabc | c <- a - b                                          |
| 7| MulI        | Opabc | c <- a * b                                          |
| 8| DivI        | Opabc | c <- a // b                                         |
| 9| AddF        | Opabc | c <- a + b                                          |
|10| SubF        | Opabc | c <- a - b                                          |
|11| MulF        | Opabc | c <- a * b                                          |
|12| DivF        | Opabc | c <- a / b                                          |
|13| EqI         | Opabc | c <- a == b                                         |
|14| NeqI        | Opabc | c <- a != b                                         |
|15| GtI         | Opabc | c <- a > b                                          |
|16| GeI         | Opabc | c <- a >= b                                         |
|16| LtI         | Opabc | c <- a < b                                          |
|17| LeI         | Opabc | c <- a <= b                                         |
|19| EqF         | Opabc | c <- a == b                                         |
|20| NeqF        | Opabc | c <- a != b                                         |
|21| GtF         | Opabc | c <- a > b                                          |
|22| GeF         | Opabc | c <- a >= b                                         |
|22| LtF         | Opabc | c <- a < b                                          |
|23| LeF         | Opabc | c <- a <= b                                         |
|24| EqB         | Opabc | c <- a == b                                         |
|25| NeqB        | Opabc | c <- a != b                                         |
|26| Copy        | OpAB  | b <- a                                              |
|27| Jmp         | OpA   | pc <- pc + a                                        |
|28| JmpIfFalse  | OpAb  | pc <- !b ? pc + a : pc                              |
|28| JmpIfTrue   | OpAb  | pc <-  b ? pc + a : pc                              |

| I| Ins         | Op    |                                                     |

## Register Usage
 - After compiling a Node, its result type, register and if it's a variable
   is stored in a structure.
    ```cpp
    struct ResultInfo {
        TypeID type;
        bool is_var = false;
        int reg = -1;
    };
    ```
 - All compile functions take an optional register to store the result.
 - If the register **is** provided:
    - The result **must** be stored in that register.
    - The given register _should_ not be passed down to any sub-expressions.
 - If the register is **not** provided:
    - A node should first compile its sub-expressions without a target
      register, and then use a free register or the child ones if possible.

