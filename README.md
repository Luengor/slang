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
```
    6         8          9           9
+--------+---------+-----------+-----------+
| OpCode |    A    |     B     |     C     |
+--------+---------+-----------+-----------+
| OpCode |    A    |           Bx          |
+--------+---------+-----------+-----------+
| OpCode |    A    |          sBx          |
+--------+---------+-----------------------+

```
- OpABC: OpCode + A + B + C
- OpABx:  OpCode + A + Bx
- OpAsBx: OpCode + A + sBx

### Instruction Reference
```
Return          return RC[B]
Self            R[A] <- self
Call            R[C] <- RO[B](R[C], ..., R[C + A - 1])
Jmp             PC <- PC + sBx 
JmpIfFalse      PC <- !R[A] ? PC + sBx : PC
JmpIfTrue       PC <-  R[A] ? PC + sBx : PC
Constant        R[A] <- C[Bx]
Object          R[A] <- O[Bx]
Closure         R[A] <- closure O[Bx]
GetUpval        R[A] <- Upval[Bx]
SetUpval        Upval[A] <- RC[Bx]
ReleaseUpval    release Upval[Bx]
Copy            R[A] <- R[Bx]
Retain          retain R[A]
Release         release R[A]
Not             R[A] <- !RC[Bx]
Negate          R[A] <- -RC[Bx]
AddIF           R[A] <- RC[B] + RC[C]
SubIF           R[A] <- RC[B] - RC[C]
MulIF           R[A] <- RC[B] * RC[C]
DivIF           R[A] <- RC[B] / RC[C]
EqIFB           R[A] <- RC[B] == RC[C]
NeIFB           R[A] <- RC[B] != RC[C]
GtIF            R[A] <- RC[B] > RC[C]
GeIF            R[A] <- RC[B] >= RC[C]
LtIF            R[A] <- RC[B] < RC[C]
LeIF            R[A] <- RC[B] <= RC[C]
Cast            R[A] <- RC[B]
```

Notes:
* Instructions with IFO suffix operate on integers, floats or booleans
  respectively.
* R\[x] denotes register x.
* PC denotes the program counter.
* C\[x] denotes constant x.
* O\[x] denotes object constant x.
* RC\[x] denotes `x < 256 ? R[x] : C[x - 256]`.
* RO\[x] denotes `x < 256 ? R[x] : O[x - 256]`.
* Call calls the function in R\[B] / O\[B - 256] with A arguments
  starting from R\[C\]. The result is stored in R\[C\].

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

