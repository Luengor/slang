auto var = 1;
auto print_var = () -> none {
    print(var);
};

# expect: 1 
print_var();

