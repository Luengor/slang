auto get_add_x = (fixed x) -> (fixed) -> fixed {
    return (fixed y) -> fixed { return x + y; };
};

auto add_1 = get_add_x(1);
auto add_2 = get_add_x(2);

# expect: 3
print(add_1(2));

# expect: 4
print(add_2(2)); 
