auto add = (fixed a, fixed b) -> fixed {
    return a + b;
};

# expect: 7
print(add(3, 4));

auto concat_check = (bool flag, fixed val) -> str {
    if (flag) return "yes";
    return "no";
};

# expect: yes
print(concat_check(true, 42));

# expect: no
print(concat_check(false, 0));
