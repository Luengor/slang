type rec_t = (fixed, fixed, rec_t) -> fixed;

auto subtract = (fixed a, fixed b, rec_t sub) -> fixed {
    if (a == 0)
        return -b;

    if (b == 0)
        return a;

    return sub(a - 1, b - 1, sub);
};

# expect: 2
print(subtract(5, 3, subtract));

# expect: -2
print(subtract(3, 5, subtract));

