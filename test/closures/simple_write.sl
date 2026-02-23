auto var = 1;
auto inc_var = () -> none {
    var = var + 1;
};

# expect: 1
print(var);

inc_var();

# expect: 2
print(var);

