# expect: 1
alias A = (B) -> none;
alias B = (A) -> none;

A fa = (B fb) -> none {};
B fb = (A fa) -> none {};

print(1);
