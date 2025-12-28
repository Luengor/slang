# slang
## Sintax
### Rules
```
program     -> declaration* EOF
declaration -> varDecl | statement
varDecl     -> ("fixed" | "float" | "bool") IDENTIFIER ( "=" expression )? ";"
statement   -> exprStmt | ifStmt | block
ifStmt      -> "if" "(" expression ")" statement ( "else" statement )?
block       -> "{" declaration* "}"
exprStmt    -> expression ";"
expression  -> assignment
assignment  -> IDENTIFIER "=" assignment | equality
equality    -> comparison ( ( "==" | "!=" ) comparison )*
comparison  -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
term        -> factor ( ( "+" | "-" | "or" ) factor )*
factor      -> unary ( ( "*" | "/" | "and" ) unary )*
unary       -> ( "-" | "not" ) unary | primary
primary     -> NUMBER | STRING | "true" | "false" | "(" expression ")" | IDENTIFIER
```

### Operators Precedence
From highest to lowest:
1. Parentheses `()`
2. Unary operators: `-`, `!`, `not`
3. Factor and AND: `*`, `/`, `and`
4. Term and OR: `+`, `-`, `or`
5. Comparison: `>`, `>=`, `<`, `<=`
6. Equality: `==`, `!=`
7. Assignment: `=`

