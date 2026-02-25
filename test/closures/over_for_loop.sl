for (fixed i = 0; i < 3; i = i + 1) {
    auto closure = () -> none { print(i + 1); };
    closure();
};

# expect: 1
# expect: 2
# expect: 3

