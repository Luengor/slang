# slang
## Sintax
### Rules
```
program     -> statement* EOF
statement   -> exprStmt
exprStmt    -> expression ";"
expression  -> equality
equality    -> comparison ( ( "==" | "!=" ) comparison )*
comparison  -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
term        -> factor ( ( "+" | "-" | "or" ) factor )*
factor      -> unary ( ( "*" | "/" | "and" ) unary )*
unary       -> ( "-" | "not" ) unary | primary
primary     -> NUMBER | STRING | "true" | "false" | "(" expression ")"
```

### Operators Precedence
From highest to lowest:
1. Parentheses `()`
2. Unary operators: `-`, `!`, `not`
3. Factor and AND: `*`, `/`, `and`
4. Term and OR: `+`, `-`, `or`
5. Comparison: `>`, `>=`, `<`, `<=`
6. Equality: `==`, `!=`

