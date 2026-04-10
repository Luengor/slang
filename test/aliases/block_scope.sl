# expect: 10
# expect: 20
# expect: 30
alias Num = fixed;

Num outer = 10;
print(outer);

{
    alias Num = float;
    Num inner = 20;
    print(inner);
}

Num outer2 = 30;
print(outer2);
