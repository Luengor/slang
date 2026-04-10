fixed i = 0;
while (i < 3) {
    i = i + 1;
    auto j = i;
    auto closure = () -> none { print(j); };
    closure();
}

# expect: 1
# expect: 2
# expect: 3

